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
#include "linphone/utils/utils.h"
#include "local-conference-event-handler-p.h"
#include "object/object-p.h"
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

// -----------------------------------------------------------------------------

static void doNotify (const string &notify, LinphoneEvent *lev) {
	LinphoneContent *content = linphone_core_create_content(lev->lc);
	linphone_content_set_buffer(content, notify.c_str(), strlen(notify.c_str()));
	linphone_event_notify(lev, content);
	linphone_content_unref(content);
	linphone_event_unref(lev);
}

// -----------------------------------------------------------------------------

void LocalConferenceEventHandlerPrivate::notifyFullState (const string &notify, LinphoneEvent *lev) {
	doNotify(notify, lev);
}

void LocalConferenceEventHandlerPrivate::notifyAllExcept (const string &notify, const Address &addr) {
	Address cleanedAddr(addr);
	cleanedAddr.setPort(0);
	for (const auto &participant : conf->getParticipants()) {
		Address cleanedParticipantAddr(participant->getAddress());
		cleanedParticipantAddr.setPort(0);
		if (participant->getPrivate()->isSubscribedToConferenceEventPackage() && !cleanedAddr.weakEqual(cleanedParticipantAddr))
			sendNotify(notify, participant->getAddress());
	}
}

void LocalConferenceEventHandlerPrivate::notifyAll (const string &notify) {
	for (const auto &participant : conf->getParticipants()) {
		if (participant->getPrivate()->isSubscribedToConferenceEventPackage())
			sendNotify(notify, participant->getAddress());
	}
}

string LocalConferenceEventHandlerPrivate::createNotify (ConferenceType confInfo) {
	lastNotify = lastNotify + 1;
	confInfo.setVersion(lastNotify);

	stringstream notify;
	Xsd::XmlSchema::NamespaceInfomap map;
	map[""].name = "urn:ietf:params:xml:ns:conference-info";
	serializeConferenceInfo(notify, confInfo, map);
	return notify.str();
}

string LocalConferenceEventHandlerPrivate::createNotifyFullState () {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	string subject = conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	confInfo.setUsers(users);
	confInfo.setConferenceDescription((const ConferenceDescriptionType) confDescr);

	for (const auto &participant : conf->getParticipants()) {
		UserType user = UserType();
		UserRolesType roles;
		UserType::EndpointSequence endpoints;
		user.setRoles(roles);
		user.setEndpoint(endpoints);
		user.setEntity(participant->getAddress().asStringUriOnly());
		user.getRoles()->getEntry().push_back(participant->isAdmin() ? "admin" : "participant");
		user.setState("full");

		for (const auto &device : participant->getPrivate()->getDevices()) {
			const string &gruu = device.getGruu().asStringUriOnly();
			EndpointType endpoint = EndpointType();
			endpoint.setEntity(gruu);
			endpoint.setState("full");
			user.getEndpoint().push_back(endpoint);
		}

		confInfo.getUsers()->getUser().push_back(user);
	}

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdded (const Address &addr) {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);
	UserType user = UserType();
	UserRolesType roles;
	UserType::EndpointSequence endpoints;

	shared_ptr<Participant> p = conf->findParticipant(addr);
	if (p) {
		for (const auto &device : p->getPrivate()->getDevices()) {
			const string &gruu = device.getGruu().asStringUriOnly();
			EndpointType endpoint = EndpointType();
			endpoint.setEntity(gruu);
			endpoint.setState("full");
			user.getEndpoint().push_back(endpoint);
		}
	}

	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState("full");

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantRemoved (const Address &addr) {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	user.setEntity(addr.asStringUriOnly());
	user.setState("deleted");
	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdmined (const Address &addr, bool isAdmin) {
	string entity = conf->getConferenceAddress().asStringUriOnly();
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

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifySubjectChanged () {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	string subject = conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	confInfo.setConferenceDescription((const ConferenceDescriptionType)confDescr);

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantDeviceAdded (const Address &addr, const Address &gruu) {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserRolesType roles;
	UserType::EndpointSequence endpoints;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState("partial");

	EndpointType endpoint = EndpointType();
	endpoint.setEntity(gruu.asStringUriOnly());
	endpoint.setState("full");
	user.getEndpoint().push_back(endpoint);

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantDeviceRemoved (const Address &addr, const Address &gruu) {
	string entity = conf->getConferenceAddress().asStringUriOnly();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserRolesType roles;
	UserType::EndpointSequence endpoints;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState("partial");

	EndpointType endpoint = EndpointType();
	endpoint.setEntity(gruu.asStringUriOnly());
	endpoint.setState("deleted");
	user.getEndpoint().push_back(endpoint);

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo);
}

void LocalConferenceEventHandlerPrivate::sendNotify (const string &notify, const Address &addr) {
	LinphoneAddress *cAddr = linphone_address_new(addr.asString().c_str());
	LinphoneEvent *lev = linphone_core_create_notify(core, cAddr, "conference");
	// Fix the From header to put the chat room URI
	lev->op->set_from(conf->getConferenceAddress().asString().c_str());
	linphone_address_unref(cAddr);
	doNotify(notify, lev);
}

// =============================================================================

LocalConferenceEventHandler::LocalConferenceEventHandler (LinphoneCore *core, LocalConference *localConf) : Object(*new LocalConferenceEventHandlerPrivate) {
	L_D();
	xercesc::XMLPlatformUtils::Initialize();
	d->conf = localConf;
	d->core = core;
	// TODO : init d->lastNotify = last notify
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
			int lastNotify = Utils::stoi(linphone_event_get_custom_header(lev, "Last-Notify-Version"));
			if(lastNotify == 0) {
				participant->getPrivate()->subscribeToConferenceEventPackage(true);
				d->notifyFullState(d->createNotifyFullState(), lev);
			} else {
				// TODO : send all missed notify from lastNotify to d->lastNotify
			}
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

void LocalConferenceEventHandler::notifyParticipantDeviceAdded (const Address &addr, const Address &gruu) {
	L_D();
	d->notifyAll(d->createNotifyParticipantDeviceAdded(addr, gruu));
}

void LocalConferenceEventHandler::notifyParticipantDeviceRemoved (const Address &addr, const Address &gruu) {
	L_D();
	d->notifyAll(d->createNotifyParticipantDeviceRemoved(addr, gruu));
}

LINPHONE_END_NAMESPACE
