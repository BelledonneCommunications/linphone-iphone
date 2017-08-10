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

#include "local-conference-event-handler.h"
#include "conference-info.hxx"
#include "private.h"
#include "object/object-p.h"

using namespace std;
using namespace conference_info;
using namespace LinphonePrivate;

class Conference::LocalConferenceEventHandlerPrivate : public ObjectPrivate {
public:
	void notifyFullState(string notify, LinphoneEvent *lev);
	void notifyAllExcept(string notify, LinphoneAddress *addr);

	LocalConference *conf;
	LinphoneCore *lc;
};

void Conference::LocalConferenceEventHandlerPrivate::notifyFullState(string notify, LinphoneEvent *lev) {
	LinphoneContent* content = linphone_core_create_content(lev->lc);
	linphone_content_set_buffer(content,notify.c_str(),strlen(notify.c_str()));
	linphone_event_notify(lev, content);
	linphone_content_unref(content);
	// linphone_event_unref(lev); ??
}

void Conference::LocalConferenceEventHandlerPrivate::notifyAllExcept(string notify, LinphoneAddress *addr) {
	for(const auto &participant : conf->getParticipants()) {
		if(!linphone_address_equal(participant.getAddress(), addr)) {
			LinphoneEvent *lev = linphone_core_create_notify(lc, participant.getAddress(), "Conference");
			LinphoneContent* content = linphone_core_create_content(lev->lc);
			linphone_content_set_buffer(content,notify.c_str(),strlen(notify.c_str()));
			linphone_event_notify(lev, content);
			linphone_content_unref(content);
			linphone_event_unref(lev);
		}
	}
}

// -------- Conference::LocalConferenceEventHandler public methods ---------
Conference::LocalConferenceEventHandler::LocalConferenceEventHandler(LinphoneCore *core, LocalConference *localConf) : Object(*new LocalConferenceEventHandlerPrivate) {
	L_D(LocalConferenceEventHandler);
	xercesc::XMLPlatformUtils::Initialize();
	d->conf = localConf;
	d->lc = core; // conf->getCore() ?
}

Conference::LocalConferenceEventHandler::~LocalConferenceEventHandler() {
	xercesc::XMLPlatformUtils::Terminate();
}

string Conference::LocalConferenceEventHandler::subscribeReceived(LinphoneEvent *lev) {
	L_D(LocalConferenceEventHandler);
	char *entity = linphone_address_as_string_uri_only(d->conf->getAddress());
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);
	stringstream notify;
	xml_schema::NamespaceInfomap map;

	map[""].name = "urn:ietf:params:xml:ns:conference-info";
	for(const auto &participant : d->conf->getParticipants()) {
		User_type user = User_type();
		User_roles_type roles;
		user.setRoles(roles);
		user.setEntity(linphone_address_as_string_uri_only(participant.getAddress()));
		user.getRoles()->getEntry().push_back(participant.isAdmin() ? "admin" : "participant");
		user.setState("full");
		confInfo.getUsers()->getUser().push_back(user);
	}

	serializeConference_info(notify, confInfo, map);
	//d->notifyFullState(notify.str(), lev);
	return notify.str();
}

string Conference::LocalConferenceEventHandler::notifyParticipantAdded(LinphoneAddress *addr) {
	L_D(LocalConferenceEventHandler);
	char *entity = linphone_address_as_string_uri_only(d->conf->getAddress());
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);
	stringstream notify;
	xml_schema::NamespaceInfomap map;

	User_type user = User_type();
	User_roles_type roles;
	user.setRoles(roles);
	user.setEntity(linphone_address_as_string_uri_only(addr));
	user.getRoles()->getEntry().push_back("participant");
	user.setState("full");
	confInfo.getUsers()->getUser().push_back(user);

	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}

string Conference::LocalConferenceEventHandler::notifyParticipantRemoved(LinphoneAddress *addr) {
	L_D(LocalConferenceEventHandler);
	char *entity = linphone_address_as_string_uri_only(d->conf->getAddress());
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);
	stringstream notify;
	xml_schema::NamespaceInfomap map;

	User_type user = User_type();
	user.setEntity(linphone_address_as_string_uri_only(addr));
	user.setState("deleted");
	confInfo.getUsers()->getUser().push_back(user);

	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}

string Conference::LocalConferenceEventHandler::notifyParticipantSetAdmin(LinphoneAddress *addr, bool isAdmin) {
	L_D(LocalConferenceEventHandler);
	char *entity = linphone_address_as_string_uri_only(d->conf->getAddress());
	Conference_type confInfo = Conference_type(entity);
	Users_type users;
	confInfo.setUsers(users);
	stringstream notify;
	xml_schema::NamespaceInfomap map;

	User_type user = User_type();
	User_roles_type roles;
	user.setRoles(roles);
	user.setEntity(linphone_address_as_string_uri_only(addr));
	user.getRoles()->getEntry().push_back(isAdmin ? "admin" : "participant");
	user.setState("partial");
	confInfo.getUsers()->getUser().push_back(user);

	serializeConference_info(notify, confInfo, map);
	//d->notifyAllExcept(notify.str(), addr);
	return notify.str();
}
