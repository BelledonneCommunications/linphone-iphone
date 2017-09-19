/*
 * local-conference-event-handler.cpp
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

#include "conference/local-conference.h"
#include "conference/participant.h"
#include "local-conference-event-handler.h"
#include "object/object-p.h"
#include "xml/conference-info.h"

#include "private.h"

// =============================================================================

using namespace std;
using namespace conference_info;

LINPHONE_BEGIN_NAMESPACE

class LocalConferenceEventHandlerPrivate : public ObjectPrivate {
public:
	void notifyFullState(string notify, LinphoneEvent *lev);
	void notifyAllExcept(string notify, const Address &addr);

	LinphoneCore *core = nullptr;
	LocalConference *conf = nullptr;
};

// -----------------------------------------------------------------------------

void LocalConferenceEventHandlerPrivate::notifyFullState(string notify, LinphoneEvent *lev) {
	LinphoneContent *content = linphone_core_create_content(lev->lc);
	linphone_content_set_buffer(content, notify.c_str(), strlen(notify.c_str()));
	linphone_event_notify(lev, content);
	linphone_content_unref(content);
	// linphone_event_unref(lev); ??
}

void LocalConferenceEventHandlerPrivate::notifyAllExcept(string notify, const Address &addr) {
	for (const auto &participant : conf->getParticipants()) {
		if (!addr.equal(participant->getAddress())) {
			LinphoneAddress *cAddr = linphone_address_new(participant->getAddress().asString().c_str());
			LinphoneEvent *lev = linphone_core_create_notify(core, cAddr, "Conference");
			linphone_address_unref(cAddr);
			LinphoneContent *content = linphone_core_create_content(lev->lc);
			linphone_content_set_buffer(content, notify.c_str(), strlen(notify.c_str()));
			linphone_event_notify(lev, content);
			linphone_content_unref(content);
			linphone_event_unref(lev);
		}
	}
}

// =============================================================================

LocalConferenceEventHandler::LocalConferenceEventHandler(LinphoneCore *core, LocalConference *localConf) : Object(*new LocalConferenceEventHandlerPrivate) {
	L_D(LocalConferenceEventHandler);
	xercesc::XMLPlatformUtils::Initialize();
	d->conf = localConf;
	d->core = core; // conf->getCore() ?
}

LocalConferenceEventHandler::~LocalConferenceEventHandler() {
	xercesc::XMLPlatformUtils::Terminate();
}

// -----------------------------------------------------------------------------

string LocalConferenceEventHandler::subscribeReceived(LinphoneEvent *lev) {
	L_D(LocalConferenceEventHandler);
	string entity = d->conf->getMe()->getAddress().asStringUriOnly();
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);
	xml_schema::NamespaceInfomap map;

	map[""].name = "urn:ietf:params:xml:ns:conference-info";
	for (const auto &participant : d->conf->getParticipants()) {
		User_type user = User_type();
		User_roles_type roles;
		user.setRoles(roles);
		user.setEntity(participant->getAddress().asStringUriOnly());
		user.getRoles()->getEntry().push_back(participant->isAdmin() ? "admin" : "participant");
		user.setState("full");
		confInfo.getUsers()->getUser().push_back(user);
	}

	stringstream notify;
	serializeConference_info(notify, confInfo, map);
	//d->notifyFullState(notify.str(), lev);
	return notify.str();
}

string LocalConferenceEventHandler::notifyParticipantAdded(const Address &addr) {
	L_D(LocalConferenceEventHandler);
	string entity = d->conf->getMe()->getAddress().asStringUriOnly();
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);

	User_type user = User_type();
	User_roles_type roles;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState("full");
	confInfo.getUsers()->getUser().push_back(user);

	xml_schema::NamespaceInfomap map;
	stringstream notify;
	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}

string LocalConferenceEventHandler::notifyParticipantRemoved(const Address &addr) {
	L_D(LocalConferenceEventHandler);
	string entity = d->conf->getMe()->getAddress().asStringUriOnly();
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);

	User_type user = User_type();
	user.setEntity(addr.asStringUriOnly());
	user.setState("deleted");
	confInfo.getUsers()->getUser().push_back(user);

	xml_schema::NamespaceInfomap map;
	stringstream notify;
	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}

string LocalConferenceEventHandler::notifyParticipantSetAdmin(const Address &addr, bool isAdmin) {
	L_D(LocalConferenceEventHandler);
	string entity = d->conf->getMe()->getAddress().asStringUriOnly();
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);

	User_type user = User_type();
	User_roles_type roles;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back(isAdmin ? "admin" : "participant");
	user.setState("partial");
	confInfo.getUsers()->getUser().push_back(user);

	xml_schema::NamespaceInfomap map;
	stringstream notify;
	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}

LINPHONE_END_NAMESPACE
