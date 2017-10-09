/*
 * local-conference-event-handler.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "conference/local-conference.h"
#include "conference/participant-p.h"
#include "local-conference-event-handler.h"
#include "object/object-p.h"

#include "private.h"

#include "xml/conference-info.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

class LocalConferenceEventHandlerPrivate : public ObjectPrivate {
public:
	void notifyFullState (string notify, LinphoneEvent *lev);
	void notifyAllExcept (string notify, const Address &addr);
	void notifyAll (string notify);
	string createNotifyFullState ();
	string createNotifyParticipantAdded (const Address &addr);
	string createNotifyParticipantRemoved (const Address &addr);
	string createNotifyParticipantAdmined (const Address &addr, bool isAdmin);
	string createNotifySubjectChanged ();

	LinphoneCore *core = nullptr;
	LocalConference *conf = nullptr;

private:
	void sendNotify (string notify, const Address &addr);
};

// -----------------------------------------------------------------------------

static void doNotify (string notify, LinphoneEvent *lev) {
	LinphoneContent *content = linphone_core_create_content(lev->lc);
	linphone_content_set_buffer(content, notify.c_str(), strlen(notify.c_str()));
	linphone_event_notify(lev, content);
	linphone_content_unref(content);
	linphone_event_unref(lev);
}

static string createNotify (ConferenceType confInfo) {
	stringstream notify;
	Xsd::XmlSchema::NamespaceInfomap map;
	map[""].name = "urn:ietf:params:xml:ns:conference-info";
	serializeConferenceInfo(notify, confInfo, map);
	return notify.str();
}

// -----------------------------------------------------------------------------

void LocalConferenceEventHandlerPrivate::notifyFullState (string notify, LinphoneEvent *lev) {
	doNotify(notify, lev);
}

void LocalConferenceEventHandlerPrivate::notifyAllExcept (string notify, const Address &addr) {
	for (const auto &participant : conf->getParticipants()) {
		if (participant->getPrivate()->isSubscribedToConferenceEventPackage() && (addr != participant->getAddress()))
			sendNotify(notify, addr);
	}
}

void LocalConferenceEventHandlerPrivate::notifyAll (string notify) {
	for (const auto &participant : conf->getParticipants()) {
		if (participant->getPrivate()->isSubscribedToConferenceEventPackage())
			sendNotify(notify, participant->getAddress());
	}
}

string LocalConferenceEventHandlerPrivate::createNotifyFullState () {
	string entity = this->conf->getConferenceAddress()->asStringUriOnly();
	string subject = this->conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	confInfo.setUsers(users);
	confInfo.setConferenceDescription((const ConferenceDescriptionType) confDescr);

	for (const auto &participant : this->conf->getParticipants()) {
		UserType user = UserType();
		UserRolesType roles;
		user.setRoles(roles);
		user.setEntity(participant->getAddress().asStringUriOnly());
		user.getRoles()->getEntry().push_back(participant->isAdmin() ? "admin" : "participant");
		user.setState("full");
		confInfo.getUsers()->getUser().push_back(user);
	}

	return(createNotify(confInfo));
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdded (const Address &addr) {
	string entity = this->conf->getConferenceAddress()->asStringUriOnly();
	string subject = this->conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserRolesType roles;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState("full");
	confInfo.getUsers()->getUser().push_back(user);

	return(createNotify(confInfo));
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantRemoved (const Address &addr) {
	string entity = this->conf->getConferenceAddress()->asStringUriOnly();
	string subject = this->conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	user.setEntity(addr.asStringUriOnly());
	user.setState("deleted");
	confInfo.getUsers()->getUser().push_back(user);

	return(createNotify(confInfo));
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdmined (const Address &addr, bool isAdmin) {
	string entity = this->conf->getConferenceAddress()->asStringUriOnly();
	string subject = this->conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserRolesType roles;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back(isAdmin ? "admin" : "participant");
	user.setState("partial");
	confInfo.getUsers()->getUser().push_back(user);

	return(createNotify(confInfo));
}

string LocalConferenceEventHandlerPrivate::createNotifySubjectChanged () {
	string entity = this->conf->getConferenceAddress()->asStringUriOnly();
	string subject = this->conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	confInfo.setConferenceDescription((const ConferenceDescriptionType)confDescr);

	return(createNotify(confInfo));
}

void LocalConferenceEventHandlerPrivate::sendNotify (string notify, const Address &addr) {
	LinphoneAddress *cAddr = linphone_address_new(addr.asString().c_str());
	LinphoneEvent *lev = linphone_core_create_notify(core, cAddr, "conference");
	linphone_address_unref(cAddr);
	doNotify(notify, lev);
}

// =============================================================================

LocalConferenceEventHandler::LocalConferenceEventHandler (LinphoneCore *core, LocalConference *localConf) : Object(*new LocalConferenceEventHandlerPrivate) {
	L_D();
	xercesc::XMLPlatformUtils::Initialize();
	d->conf = localConf;
	d->core = core;
}

LocalConferenceEventHandler::~LocalConferenceEventHandler () {
	xercesc::XMLPlatformUtils::Terminate();
}

// -----------------------------------------------------------------------------

void LocalConferenceEventHandler::subscribeReceived (LinphoneEvent *lev) {
	L_D();
	const LinphoneAddress *lAddr = linphone_event_get_from(lev);
	char *addrStr = linphone_address_as_string(lAddr);
	shared_ptr<Participant> participant = d->conf->findParticipant(Address(addrStr));
	bctbx_free(addrStr);
	if (participant) {
		if (linphone_event_get_subscription_state(lev) == LinphoneSubscriptionActive) {
			participant->getPrivate()->subscribeToConferenceEventPackage(true);
			d->notifyFullState(d->createNotifyFullState(), lev);
		} else if (linphone_event_get_subscription_state(lev) == LinphoneSubscriptionTerminated)
			participant->getPrivate()->subscribeToConferenceEventPackage(false);
	}
}

void LocalConferenceEventHandler::notifyParticipantAdded (const Address &addr) {
	L_D();
	d->notifyAllExcept(d->createNotifyParticipantAdded(addr), addr);
}

void LocalConferenceEventHandler::notifyParticipantRemoved (const Address &addr) {
	L_D();
	d->notifyAllExcept(d->createNotifyParticipantRemoved(addr), addr);
}

void LocalConferenceEventHandler::notifyParticipantSetAdmin (const Address &addr, bool isAdmin) {
	L_D();
	d->notifyAll(d->createNotifyParticipantAdmined(addr, isAdmin));
}

void LocalConferenceEventHandler::notifySubjectChanged () {
	L_D();
	d->notifyAll(d->createNotifySubjectChanged());
}

LINPHONE_END_NAMESPACE
