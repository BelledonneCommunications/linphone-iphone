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

#include <map>
#include <string>

#include "address/identity-address.h"
#include "conference/conference-listener.h"
#include "conference/handlers/local-conference-event-handler-p.h"
#include "conference/handlers/remote-conference-event-handler-p.h"
#include "conference/local-conference-p.h"
#include "conference/local-conference.h"
#include "conference/participant-p.h"
#include "conference/remote-conference.h"
#include "liblinphone_tester.h"
#include "linphone/core.h"
#include "private.h"
#include "tester_utils.h"
#include "tools/private-access.h"
#include "tools/tester.h"

using namespace LinphonePrivate;
using namespace std;

static const char *first_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"full\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:bob@example.com\" state=\"full\">"\
"      <display-text>Bob Hoskins</display-text>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:bob@pc33.example.com\">"\
"       <display-text>Bob's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"   <!--"\
"     USER"\
"   -->"\
"     <user entity=\"sip:alice@example.com\" state=\"full\">"\
"      <display-text>Alice</display-text>"\
"      <roles>"\
"      	<entry>admin</entry>"\
"      	<entry>participant</entry>"\
"      </roles>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:4kfk4j392jsu@example.com;grid=433kj4j3u\">"\
"       <status>connected</status>"\
"       <joining-method>dialed-out</joining-method>"\
"       <joining-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <by>sip:mike@example.com</by>"\
"       </joining-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>534232</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"      <endpoint entity=\"sip:aliced48ed45@example.com;grid=54def54e8\">"\
"       <status>connected</status>"\
"       <joining-method>dialed-out</joining-method>"\
"       <joining-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <by>sip:mike@example.com</by>"\
"       </joining-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>534232</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *participant_added_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"partial\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:frank@example.com\" state=\"full\">"\
"      <display-text>Bob Hoskins</display-text>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:frank@pc33.example.com\">"\
"       <display-text>Frank's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *participant_not_added_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"partial\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:frank@example.com\" state=\"partial\">"\
"      <display-text>Bob Hoskins</display-text>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:frank@pc33.example.com\">"\
"       <display-text>Frank's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *participant_deleted_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"partial\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:bob@example.com\" state=\"deleted\">"\
"      <display-text>Bob Hoskins</display-text>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:bob@pc33.example.com\">"\
"       <display-text>Bob's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *participant_admined_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"partial\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:bob@example.com\" state=\"partial\">"\
"      <display-text>Bob Hoskins</display-text>"\
"      <roles>"\
"      	<entry>participant</entry>"\
"      	<entry>admin</entry>"\
"      </roles>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:bob@pc33.example.com\">"\
"       <display-text>Bob's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *participant_unadmined_notify = \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> "\
"   <conference-info"\
"    xmlns=\"urn:ietf:params:xml:ns:conference-info\""\
"    entity=\"%s\""\
"    state=\"partial\" version=\"1\">"\
"   <!--"\
"     CONFERENCE INFO"\
"   -->"\
"    <conference-description>"\
"     <subject>Agenda: This month's goals</subject>"\
"      <service-uris>"\
"       <entry>"\
"        <uri>http://sharepoint/salesgroup/</uri>"\
"        <purpose>web-page</purpose>"\
"       </entry>"\
"      </service-uris>"\
"     </conference-description>"\
"   <!--"\
"      CONFERENCE STATE"\
"   -->"\
"    <conference-state>"\
"     <user-count>33</user-count>"\
"    </conference-state>"\
"   <!--"\
"     USERS"\
"   -->"\
"    <users>"\
"     <user entity=\"sip:alice@example.com\" state=\"partial\">"\
"      <display-text>Alice Hoskins</display-text>"\
"      <roles>"\
"      	<entry>participant</entry>"\
"      </roles>"\
"   <!--"\
"     ENDPOINTS"\
"   -->"\
"      <endpoint entity=\"sip:alice@pc33.example.com\">"\
"       <display-text>Alice's Laptop</display-text>"\
"       <status>disconnected</status>"\
"       <disconnection-method>departed</disconnection-method>"\
"       <disconnection-info>"\
"        <when>2005-03-04T20:00:00Z</when>"\
"        <reason>bad voice quality</reason>"\
"        <by>sip:mike@example.com</by>"\
"       </disconnection-info>"\
"   <!--"\
"     MEDIA"\
"   -->"\
"       <media id=\"1\">"\
"        <display-text>main audio</display-text>"\
"        <type>audio</type>"\
"        <label>34567</label>"\
"        <src-id>432424</src-id>"\
"        <status>sendrecv</status>"\
"       </media>"\
"      </endpoint>"\
"     </user>"\
"    </users>"\
"   </conference-info>";

static const char *bobUri = "sip:bob@example.com";
static const char *aliceUri = "sip:alice@example.com";
static const char *frankUri = "sip:frank@example.com";
static const char *confUri = "sips:conf233@example.com";

L_ENABLE_ATTR_ACCESS(LocalConferencePrivate, unique_ptr<LocalConferenceEventHandler>, eventHandler);

class ConferenceEventTester : public RemoteConference {
public:
	ConferenceEventTester (const shared_ptr<Core> &core, const Address &confAddr);
	~ConferenceEventTester ();

private:
	void onConferenceCreated (const IdentityAddress &addr) override;
	void onConferenceKeywordsChanged (const vector<string> &keywords) override;
	void onConferenceTerminated (const IdentityAddress &addr) override;
	void onFirstNotifyReceived (const IdentityAddress &addr) override;
	void onParticipantAdded (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onParticipantRemoved (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onParticipantSetAdmin (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) override;
	void onSubjectChanged (const shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) override;
	void onParticipantDeviceAdded (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) override;
	void onParticipantDeviceRemoved (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) override;

public:
	RemoteConferenceEventHandler *handler;
	map<string, bool> participants;
	map<string, int> participantDevices;
	string confSubject;
	bool oneToOne = false;
};

ConferenceEventTester::ConferenceEventTester (const shared_ptr<Core> &core, const Address &confAddr)
	: RemoteConference(core, confAddr, nullptr) {
	handler = new RemoteConferenceEventHandler(this);
}

ConferenceEventTester::~ConferenceEventTester () {
	delete handler;
}

void ConferenceEventTester::onConferenceCreated (const IdentityAddress &addr) {}

void ConferenceEventTester::onConferenceKeywordsChanged (const vector<string> &keywords) {
	for (const auto &k : keywords) {
		if (k == "one-to-one")
			oneToOne = true;
	}
}

void ConferenceEventTester::onConferenceTerminated (const IdentityAddress &addr) {}

void ConferenceEventTester::onFirstNotifyReceived (const IdentityAddress &addr) {}

void ConferenceEventTester::onParticipantAdded (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	const IdentityAddress addr = event->getParticipantAddress();
	participants.insert({ addr.asString(), false });
	participantDevices.insert({ addr.asString(), 0 });
}

void ConferenceEventTester::onParticipantRemoved (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	const IdentityAddress addr = event->getParticipantAddress();
	participants.erase(addr.asString());
	participantDevices.erase(addr.asString());
}

void ConferenceEventTester::onParticipantSetAdmin (const shared_ptr<ConferenceParticipantEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	const IdentityAddress addr = event->getParticipantAddress();
	auto it = participants.find(addr.asString());
	if (it != participants.end())
		it->second = (event->getType() == EventLog::Type::ConferenceParticipantSetAdmin);
}

void ConferenceEventTester::onSubjectChanged(const shared_ptr<ConferenceSubjectEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	confSubject = event->getSubject();
}

void ConferenceEventTester::onParticipantDeviceAdded (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	const IdentityAddress addr = event->getParticipantAddress();
	auto it = participantDevices.find(addr.asString());
	if (it != participantDevices.end())
		it->second++;

}

void ConferenceEventTester::onParticipantDeviceRemoved (const shared_ptr<ConferenceParticipantDeviceEvent> &event, bool isFullState) {
	(void)isFullState; // unused
	const IdentityAddress addr = event->getParticipantAddress();
	auto it = participantDevices.find(addr.asString());
	if (it != participantDevices.end() && it->second > 0)
		it->second--;
}

void first_notify_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;

	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_STRING_EQUAL(tester->confSubject.c_str(), "Agenda: This month's goals");
	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(linphone_address_as_string(bobAddr)) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(linphone_address_as_string(aliceAddr)) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(linphone_address_as_string(bobAddr))->second, 1, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(linphone_address_as_string(aliceAddr))->second, 2, int, "%d");

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void first_notify_parsing_wrong_conf() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, "sips:conf322@example.com");
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 0, int, "%d");
	BC_ASSERT_FALSE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_FALSE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void participant_added_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	LinphoneAddress *frankAddr = linphone_core_interpret_url(marie->lc, frankUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];
	size_t size2 = strlen(participant_added_notify) + strlen(confUri);
	char *notify_added = new char[size2];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	snprintf(notify_added, size2, participant_added_notify, confUri);
	tester->handler->notifyReceived(notify_added);

	delete[] notify_added;

	BC_ASSERT_EQUAL(tester->participants.size(), 3, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 3, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(frankAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(frankAddr))->second);

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	linphone_address_unref(frankAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void participant_not_added_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	LinphoneAddress *frankAddr = linphone_core_interpret_url(marie->lc, frankUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];
	size_t size2 = strlen(participant_not_added_notify) + strlen(confUri);
	char *notify_not_added = new char[size2];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	snprintf(notify_not_added, size2, participant_not_added_notify, confUri);
	tester->handler->notifyReceived(notify_not_added);

	delete[] notify_not_added;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_FALSE(tester->participants.find(linphone_address_as_string(frankAddr)) != tester->participants.end());

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	linphone_address_unref(frankAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void participant_deleted_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];
	size_t size2 = strlen(participant_deleted_notify) + strlen(confUri);
	char *notify_deleted = new char[size2];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	snprintf(notify_deleted, size2, participant_deleted_notify, confUri);
	tester->handler->notifyReceived(notify_deleted);

	delete[] notify_deleted;

	BC_ASSERT_EQUAL(tester->participants.size(), 1, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 1, int, "%d");
	BC_ASSERT_FALSE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void participant_admined_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];
	size_t size2 = strlen(participant_admined_notify) + strlen(confUri);
	char *notify_admined = new char[size2];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	snprintf(notify_admined, size2, participant_admined_notify, confUri);
	tester->handler->notifyReceived(notify_admined);

	delete[] notify_admined;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr))->second);

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void participant_unadmined_parsing() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *confAddress = linphone_core_interpret_url(marie->lc, confUri);
	char *confAddressStr = linphone_address_as_string(confAddress);
	Address addr(confAddressStr);
	bctbx_free(confAddressStr);
	linphone_address_unref(confAddress);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	LinphoneAddress *bobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	LinphoneAddress *aliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	size_t size = strlen(first_notify) + strlen(confUri);
	char *notify = new char[size];
	size_t size2 = strlen(participant_unadmined_notify) + strlen(confUri);
	char *notify_unadmined = new char[size2];

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	snprintf(notify, size, first_notify, confUri);
	tester->handler->notifyReceived(notify);

	delete[] notify;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(bobAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(bobAddr))->second);
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	snprintf(notify_unadmined, size2, participant_unadmined_notify, confUri);
	tester->handler->notifyReceived(notify_unadmined);

	delete[] notify_unadmined;

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(linphone_address_as_string(aliceAddr)) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(linphone_address_as_string(aliceAddr))->second);

	linphone_address_unref(bobAddr);
	linphone_address_unref(aliceAddr);
	tester = nullptr;
	linphone_core_manager_destroy(marie);
}

void send_first_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	localConf->setSubject("A random test subject");
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_STRING_EQUAL(tester->confSubject.c_str(), "A random test subject");
	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_added_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);
	LinphoneAddress *cFrankAddr = linphone_core_interpret_url(marie->lc, frankUri);
	char *frankAddrStr = linphone_address_as_string(cFrankAddr);
	Address frankAddr(frankAddrStr);
	bctbx_free(frankAddrStr);
	linphone_address_unref(cFrankAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantAdded(frankAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 3, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 3, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(frankAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);
	BC_ASSERT_TRUE(!tester->participants.find(frankAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_removed_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantRemoved(bobAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 1, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.size(), 1, int, "%d");
	BC_ASSERT_FALSE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_admined_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantAdminStatusChanged(bobAddr, true);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_unadmined_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantAdminStatusChanged(aliceAddr, false);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(aliceAddr.asString())->second);
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_subject_changed_notify () {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	localConf->setSubject("A random test subject");
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_STRING_EQUAL(tester->confSubject.c_str(), "A random test subject");
	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->handler->getLastNotify(), 1, int, "%d");
	BC_ASSERT_EQUAL(localHandlerPrivate->getLastNotify(), 1, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	localConf->setSubject("Another random test subject...");
	notify = localHandlerPrivate->createNotifySubjectChanged();
	tester->handler->notifyReceived(notify);

	BC_ASSERT_STRING_EQUAL(tester->confSubject.c_str(), "Another random test subject...");
	BC_ASSERT_EQUAL(tester->participants.size(), 2, int, "%d");
	BC_ASSERT_EQUAL(tester->handler->getLastNotify(), 2, int, "%d");
	BC_ASSERT_EQUAL(localHandlerPrivate->getLastNotify(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participants.find(bobAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString()) != tester->participants.end());
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_device_added_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantDeviceAdded(aliceAddr, aliceAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 1, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void send_device_removed_notify() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);
	LinphoneAddress *cAliceAddr = linphone_core_interpret_url(marie->lc, aliceUri);
	char *aliceAddrStr = linphone_address_as_string(cAliceAddr);
	Address aliceAddr(aliceAddrStr);
	bctbx_free(aliceAddrStr);
	linphone_address_unref(cAliceAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	localConf->addParticipant(aliceAddr, &params, false);
	localConf->setSubject("A random test subject");
	shared_ptr<Participant> alice = localConf->findParticipant(aliceAddr);
	L_GET_PRIVATE(alice)->setAdmin(true);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState();

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantDeviceAdded(aliceAddr, aliceAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 1, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantDeviceRemoved(aliceAddr, aliceAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	notify = localHandlerPrivate->createNotifyParticipantDeviceRemoved(aliceAddr, aliceAddr);
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 2, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_TRUE(tester->participantDevices.find(aliceAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_EQUAL(tester->participantDevices.find(aliceAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_TRUE(!tester->participants.find(bobAddr.asString())->second);
	BC_ASSERT_TRUE(tester->participants.find(aliceAddr.asString())->second);

	tester = nullptr;
	localConf = nullptr;
	alice = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void one_to_one_keyword () {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	char *identityStr = linphone_address_as_string(pauline->identity);
	Address addr(identityStr);
	bctbx_free(identityStr);
	shared_ptr<ConferenceEventTester> tester = make_shared<ConferenceEventTester>(marie->lc->cppPtr, addr);
	shared_ptr<LocalConference> localConf = make_shared<LocalConference>(pauline->lc->cppPtr, addr, nullptr);
	LinphoneAddress *cBobAddr = linphone_core_interpret_url(marie->lc, bobUri);
	char *bobAddrStr = linphone_address_as_string(cBobAddr);
	Address bobAddr(bobAddrStr);
	bctbx_free(bobAddrStr);
	linphone_address_unref(cBobAddr);

	CallSessionParams params;
	localConf->addParticipant(bobAddr, &params, false);
	LocalConferenceEventHandlerPrivate *localHandlerPrivate = L_GET_PRIVATE(
		L_ATTR_GET(L_GET_PRIVATE(localConf), eventHandler)
	);
	const_cast<IdentityAddress &>(localConf->getConferenceAddress()) = addr;
	string notify = localHandlerPrivate->createNotifyFullState(-1, true);

	const_cast<IdentityAddress &>(tester->handler->getChatRoomId().getPeerAddress()) = addr;
	tester->handler->notifyReceived(notify);

	BC_ASSERT_EQUAL(tester->participantDevices.size(), 1, int, "%d");
	BC_ASSERT_TRUE(tester->participantDevices.find(bobAddr.asString()) != tester->participantDevices.end());
	BC_ASSERT_EQUAL(tester->participantDevices.find(bobAddr.asString())->second, 0, int, "%d");
	BC_ASSERT_TRUE(tester->oneToOne);

	tester = nullptr;
	localConf = nullptr;
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t conference_event_tests[] = {
	TEST_NO_TAG("First notify parsing", first_notify_parsing),
	TEST_NO_TAG("First notify parsing wrong conf", first_notify_parsing_wrong_conf),
	TEST_NO_TAG("Participant added", participant_added_parsing),
	TEST_NO_TAG("Participant not added", participant_not_added_parsing),
	TEST_NO_TAG("Participant deleted", participant_deleted_parsing),
	TEST_NO_TAG("Participant admined", participant_admined_parsing),
	TEST_NO_TAG("Participant unadmined", participant_unadmined_parsing),
	TEST_NO_TAG("Send first notify", send_first_notify),
	TEST_NO_TAG("Send participant added notify", send_added_notify),
	TEST_NO_TAG("Send participant removed notify", send_removed_notify),
	TEST_NO_TAG("Send participant admined notify", send_admined_notify),
	TEST_NO_TAG("Send participant unadmined notify", send_unadmined_notify),
	TEST_NO_TAG("Send subject changed notify", send_subject_changed_notify),
	TEST_NO_TAG("Send device added notify", send_device_added_notify),
	TEST_NO_TAG("Send device removed notify", send_device_removed_notify),
	TEST_NO_TAG("one-to-one keyword", one_to_one_keyword)
};

test_suite_t conference_event_test_suite = {
	"Conference event",
	nullptr,
	nullptr,
	liblinphone_tester_before_each,
	liblinphone_tester_after_each,
	sizeof(conference_event_tests) / sizeof(conference_event_tests[0]), conference_event_tests
};
