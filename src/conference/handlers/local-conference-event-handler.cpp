/*
 * local-conference-event-handler.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
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

#include <ctime>

#include "linphone/api/c-content.h"
#include "linphone/utils/utils.h"

#include "c-wrapper/c-wrapper.h"
#include "conference/local-conference.h"
#include "conference/participant-device.h"
#include "conference/participant-p.h"
#include "content/content-manager.h"
#include "content/content-type.h"
#include "content/content.h"
#include "core/core-p.h"
#include "db/main-db.h"
#include "event-log/events.h"
#include "local-conference-event-handler-p.h"
#include "logger/logger.h"
#include "object/object-p.h"

// TODO: remove me.
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

// -----------------------------------------------------------------------------

void LocalConferenceEventHandlerPrivate::notifyFullState (const string &notify, const shared_ptr<ParticipantDevice> &device) {
	notifyParticipantDevice(notify, device);
}

void LocalConferenceEventHandlerPrivate::notifyAllExcept (const string &notify, const shared_ptr<Participant> &exceptParticipant) {
	for (const auto &participant : conf->getParticipants()) {
		if (participant != exceptParticipant)
			notifyParticipant(notify, participant);
	}
}

void LocalConferenceEventHandlerPrivate::notifyAll (const string &notify) {
	for (const auto &participant : conf->getParticipants())
		notifyParticipant(notify, participant);
}

string LocalConferenceEventHandlerPrivate::createNotifyFullState (int notifyId, bool oneToOne) {
	string entity = conf->getConferenceAddress().asString();
	string subject = conf->getSubject();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	if (oneToOne) {
		KeywordsType keywords(sizeof(char), "one-to-one");
		confDescr.setKeywords(keywords);
	}
	confInfo.setUsers(users);
	confInfo.setConferenceDescription((const ConferenceDescriptionType) confDescr);

	for (const auto &participant : conf->getParticipants()) {
		UserType user = UserType();
		UserRolesType roles;
		UserType::EndpointSequence endpoints;
		user.setRoles(roles);
		user.setEndpoint(endpoints);
		user.setEntity(participant->getAddress().asString());
		user.getRoles()->getEntry().push_back(participant->isAdmin() ? "admin" : "participant");
		user.setState(StateType::full);

		for (const auto &device : participant->getPrivate()->getDevices()) {
			const string &gruu = device->getAddress().asString();
			EndpointType endpoint = EndpointType();
			endpoint.setEntity(gruu);
			endpoint.setState(StateType::full);
			user.getEndpoint().push_back(endpoint);
		}

		confInfo.getUsers()->getUser().push_back(user);
	}

	return createNotify(confInfo, notifyId, true);
}

string LocalConferenceEventHandlerPrivate::createNotifyMultipart (int notifyId) {
	list<shared_ptr<EventLog>> events = conf->getCore()->getPrivate()->mainDb->getConferenceNotifiedEvents(
		ChatRoomId(conf->getConferenceAddress(), conf->getConferenceAddress()),
		static_cast<unsigned int>(notifyId)
	);

	list<Content> contents;
	for (const auto &eventLog : events) {
		Content *content = new Content();
		content->setContentType(ContentType::ConferenceInfo);
		string body;
		shared_ptr<ConferenceNotifiedEvent> notifiedEvent = static_pointer_cast<ConferenceNotifiedEvent>(eventLog);
		int eventNotifyId = static_cast<int>(notifiedEvent->getNotifyId());
		switch (eventLog->getType()) {
			case EventLog::Type::ConferenceParticipantAdded: {
				shared_ptr<ConferenceParticipantEvent> addedEvent = static_pointer_cast<ConferenceParticipantEvent>(eventLog);
				body = createNotifyParticipantAdded(
					addedEvent->getParticipantAddress(),
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceParticipantRemoved: {
				shared_ptr<ConferenceParticipantEvent> removedEvent = static_pointer_cast<ConferenceParticipantEvent>(eventLog);
				body = createNotifyParticipantRemoved(
					removedEvent->getParticipantAddress(),
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceParticipantSetAdmin: {
				shared_ptr<ConferenceParticipantEvent> setAdminEvent = static_pointer_cast<ConferenceParticipantEvent>(eventLog);
				body = createNotifyParticipantAdminStatusChanged(
					setAdminEvent->getParticipantAddress(),
					true,
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceParticipantUnsetAdmin: {
				shared_ptr<ConferenceParticipantEvent> unsetAdminEvent = static_pointer_cast<ConferenceParticipantEvent>(eventLog);
				body = createNotifyParticipantAdminStatusChanged(
					unsetAdminEvent->getParticipantAddress(),
					false,
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceParticipantDeviceAdded: {
				shared_ptr<ConferenceParticipantDeviceEvent> deviceAddedEvent = static_pointer_cast<ConferenceParticipantDeviceEvent>(eventLog);
				body = createNotifyParticipantDeviceAdded(
					deviceAddedEvent->getParticipantAddress(),
					deviceAddedEvent->getDeviceAddress(),
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceParticipantDeviceRemoved: {
				shared_ptr<ConferenceParticipantDeviceEvent> deviceRemovedEvent = static_pointer_cast<ConferenceParticipantDeviceEvent>(eventLog);
				body = createNotifyParticipantDeviceRemoved(
					deviceRemovedEvent->getParticipantAddress(),
					deviceRemovedEvent->getDeviceAddress(),
					eventNotifyId
				);
			} break;

			case EventLog::Type::ConferenceSubjectChanged: {
				shared_ptr<ConferenceSubjectEvent> subjectEvent = static_pointer_cast<ConferenceSubjectEvent>(eventLog);
				body = createNotifySubjectChanged(
					subjectEvent->getSubject(),
					eventNotifyId
				);
			} break;

			default:
				// We should never pass here!
				L_ASSERT(false);
				continue;
		}
		contents.emplace_back(Content());
		contents.back().setContentType(ContentType::ConferenceInfo);
		contents.back().setBody(body);
	}

	if (contents.empty())
		return Utils::getEmptyConstRefObject<string>();

	list<Content *> contentPtrs;
	for (auto &content : contents)
		contentPtrs.push_back(&content);
	string multipart = ContentManager::contentListToMultipart(contentPtrs).getBodyAsString();
	return multipart;
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdded (const Address &addr, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);
	UserType user = UserType();
	UserRolesType roles;
	UserType::EndpointSequence endpoints;

	shared_ptr<Participant> p = conf->findParticipant(addr);
	if (p) {
		for (const auto &device : p->getPrivate()->getDevices()) {
			const string &gruu = device->getAddress().asString();
			EndpointType endpoint = EndpointType();
			endpoint.setEntity(gruu);
			endpoint.setState(StateType::full);
			user.getEndpoint().push_back(endpoint);
		}
	}

	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back("participant");
	user.setState(StateType::full);

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo, notifyId);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantAdminStatusChanged (const Address &addr, bool isAdmin, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserRolesType roles;
	user.setRoles(roles);
	user.setEntity(addr.asStringUriOnly());
	user.getRoles()->getEntry().push_back(isAdmin ? "admin" : "participant");
	user.setState(StateType::partial);
	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo, notifyId);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantRemoved (const Address &addr, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	user.setEntity(addr.asStringUriOnly());
	user.setState(StateType::deleted);
	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo, notifyId);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantDeviceAdded (const Address &addr, const Address &gruu, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserType::EndpointSequence endpoints;
	user.setEntity(addr.asStringUriOnly());
	user.setState(StateType::partial);

	EndpointType endpoint = EndpointType();
	endpoint.setEntity(gruu.asStringUriOnly());
	endpoint.setState(StateType::full);
	user.getEndpoint().push_back(endpoint);

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo, notifyId);
}

string LocalConferenceEventHandlerPrivate::createNotifyParticipantDeviceRemoved (const Address &addr, const Address &gruu, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	UsersType users;
	confInfo.setUsers(users);

	UserType user = UserType();
	UserType::EndpointSequence endpoints;
	user.setEntity(addr.asStringUriOnly());
	user.setState(StateType::partial);

	EndpointType endpoint = EndpointType();
	endpoint.setEntity(gruu.asStringUriOnly());
	endpoint.setState(StateType::deleted);
	user.getEndpoint().push_back(endpoint);

	confInfo.getUsers()->getUser().push_back(user);

	return createNotify(confInfo, notifyId);
}

string LocalConferenceEventHandlerPrivate::createNotifySubjectChanged (int notifyId) {
	return createNotifySubjectChanged(conf->getSubject(), notifyId);
}

// -----------------------------------------------------------------------------

void LocalConferenceEventHandlerPrivate::notifyResponseCb (const LinphoneEvent *ev) {
	LinphoneEventCbs *cbs = linphone_event_get_callbacks(ev);
	LocalConferenceEventHandlerPrivate *handler = reinterpret_cast<LocalConferenceEventHandlerPrivate *>(
		linphone_event_cbs_get_user_data(cbs)
	);
	linphone_event_cbs_set_user_data(cbs, nullptr);
	linphone_event_cbs_set_notify_response(cbs, nullptr);

	if (linphone_event_get_reason(ev) != LinphoneReasonNone)
		return;

	for (const auto &p : handler->conf->getParticipants()) {
		for (const auto &d : p->getPrivate()->getDevices()) {
			if ((d->getConferenceSubscribeEvent() == ev) && (d->getState() == ParticipantDevice::State::Joining)) {
				handler->conf->onFirstNotifyReceived(d->getAddress());
				return;
			}
		}
	}
}

// -----------------------------------------------------------------------------

string LocalConferenceEventHandlerPrivate::createNotify (ConferenceType confInfo, int notifyId, bool isFullState) {
	confInfo.setVersion(notifyId == -1 ? ++lastNotify : static_cast<unsigned int>(notifyId));
	confInfo.setState(isFullState ? StateType::full : StateType::partial);

	if (!confInfo.getConferenceDescription()) {
		ConferenceDescriptionType description = ConferenceDescriptionType();
		confInfo.setConferenceDescription(description);
	}

	time_t result = time(nullptr);
	confInfo.getConferenceDescription()->setFreeText(Utils::toString(static_cast<long>(result)));

	stringstream notify;
	Xsd::XmlSchema::NamespaceInfomap map;
	map[""].name = "urn:ietf:params:xml:ns:conference-info";
	serializeConferenceInfo(notify, confInfo, map);
	return notify.str();
}

string LocalConferenceEventHandlerPrivate::createNotifySubjectChanged (const string &subject, int notifyId) {
	string entity = conf->getConferenceAddress().asString();
	ConferenceType confInfo = ConferenceType(entity);
	ConferenceDescriptionType confDescr = ConferenceDescriptionType();
	confDescr.setSubject(subject);
	confInfo.setConferenceDescription((const ConferenceDescriptionType)confDescr);

	return createNotify(confInfo, notifyId);
}

void LocalConferenceEventHandlerPrivate::notifyParticipant (const string &notify, const shared_ptr<Participant> &participant) {
	for (const auto &device : participant->getPrivate()->getDevices())
		notifyParticipantDevice(notify, device);
}

void LocalConferenceEventHandlerPrivate::notifyParticipantDevice (const string &notify, const shared_ptr<ParticipantDevice> &device, bool multipart) {
	if (!device->isSubscribedToConferenceEventPackage() || notify.empty())
		return;

	LinphoneEvent *ev = device->getConferenceSubscribeEvent();
	LinphoneEventCbs *cbs = linphone_event_get_callbacks(ev);
	linphone_event_cbs_set_user_data(cbs, this);
	linphone_event_cbs_set_notify_response(cbs, notifyResponseCb);

	Content content;
	content.setBody(notify);
	ContentType contentType;
	if (multipart) {
		contentType = ContentType(ContentType::Multipart);
		contentType.addParameter("boundary", MultipartBoundary);
	} else
		contentType = ContentType(ContentType::ConferenceInfo);

	content.setContentType(contentType);
	// TODO: Activate compression
	//if (linphone_core_content_encoding_supported(conf->getCore()->getCCore(), "deflate"))
	//	linphone_content_set_encoding(content, "deflate");
	// TODO: Activate compression
	LinphoneContent *cContent = L_GET_C_BACK_PTR(&content);
	linphone_event_notify(ev, cContent);
}

// =============================================================================

LocalConferenceEventHandler::LocalConferenceEventHandler (LocalConference *localConference, unsigned int notify) :
	Object(*new LocalConferenceEventHandlerPrivate) {
	L_D();
	d->conf = localConference;
	d->lastNotify = notify;
}

// -----------------------------------------------------------------------------

void LocalConferenceEventHandler::subscribeReceived (LinphoneEvent *lev, bool oneToOne) {
	L_D();
	const LinphoneAddress *lAddr = linphone_event_get_from(lev);
	char *addrStr = linphone_address_as_string(lAddr);
	shared_ptr<Participant> participant = d->conf->findParticipant(Address(addrStr));
	bctbx_free(addrStr);
	if (!participant) {
		lError() << "received SUBSCRIBE corresponds to no participant of the conference; " << d->conf->getConferenceAddress().asString() << ", no NOTIFY sent.";
		linphone_event_deny_subscription(lev, LinphoneReasonDeclined);
		return;
	}

	const LinphoneAddress *lContactAddr = linphone_event_get_remote_contact(lev);
	char *contactAddrStr = linphone_address_as_string(lContactAddr);
	IdentityAddress contactAddr(contactAddrStr);
	bctbx_free(contactAddrStr);
	shared_ptr<ParticipantDevice> device = participant->getPrivate()->findDevice(contactAddr);
	if (!device || (device->getState() != ParticipantDevice::State::Present && device->getState() != ParticipantDevice::State::Joining)) {
		lError() << "received SUBSCRIBE for conference: " << d->conf->getConferenceAddress().asString()
			<< "device sending subscribe: " << contactAddr.asString() << " is not known, no NOTIFY sent.";
		linphone_event_deny_subscription(lev, LinphoneReasonDeclined);
		return;
	}

	linphone_event_accept_subscription(lev);
	if (linphone_event_get_subscription_state(lev) == LinphoneSubscriptionActive) {
		unsigned int lastNotify = static_cast<unsigned int>(Utils::stoi(linphone_event_get_custom_header(lev, "Last-Notify-Version")));
		device->setConferenceSubscribeEvent(lev);
		if (lastNotify == 0 || (device->getState() == ParticipantDevice::State::Joining)) {
			lInfo() << "Sending initial notify of conference:" << d->conf->getConferenceAddress().asString() << " to: " << device->getAddress().asString();
			d->notifyFullState(d->createNotifyFullState(static_cast<int>(d->lastNotify), oneToOne), device);
		} else if (lastNotify < d->lastNotify) {
			lInfo() << "Sending all missed notify [" << lastNotify << "-" << d->lastNotify <<
				"] for conference:" << d->conf->getConferenceAddress().asString() <<
				" to: " << participant->getAddress().asString();
			d->notifyParticipantDevice(d->createNotifyMultipart(static_cast<int>(lastNotify)), device, true);
		} else if (lastNotify > d->lastNotify) {
			lError() << "last notify received by client: [" << lastNotify <<"] for conference:" <<
				d->conf->getConferenceAddress().asString() <<
				" should not be higher than last notify sent by server: [" << d->lastNotify << "]";
		}
	} else if (linphone_event_get_subscription_state(lev) == LinphoneSubscriptionTerminated)
		device->setConferenceSubscribeEvent(nullptr);
}

shared_ptr<ConferenceParticipantEvent> LocalConferenceEventHandler::notifyParticipantAdded (const Address &addr) {
	L_D();
	shared_ptr<Participant> participant = d->conf->findParticipant(addr);
	d->notifyAllExcept(d->createNotifyParticipantAdded(addr), participant);
	shared_ptr<ConferenceParticipantEvent> event = make_shared<ConferenceParticipantEvent>(
		EventLog::Type::ConferenceParticipantAdded,
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		addr
	);
	return event;
}

shared_ptr<ConferenceParticipantEvent> LocalConferenceEventHandler::notifyParticipantRemoved (const Address &addr) {
	L_D();
	shared_ptr<Participant> participant = d->conf->findParticipant(addr);
	d->notifyAllExcept(d->createNotifyParticipantRemoved(addr), participant);
	shared_ptr<ConferenceParticipantEvent> event = make_shared<ConferenceParticipantEvent>(
		EventLog::Type::ConferenceParticipantRemoved,
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		addr
	);
	return event;
}

shared_ptr<ConferenceParticipantEvent> LocalConferenceEventHandler::notifyParticipantSetAdmin (const Address &addr, bool isAdmin) {
	L_D();
	d->notifyAll(d->createNotifyParticipantAdminStatusChanged(addr, isAdmin));
	shared_ptr<ConferenceParticipantEvent> event = make_shared<ConferenceParticipantEvent>(
		isAdmin ? EventLog::Type::ConferenceParticipantSetAdmin : EventLog::Type::ConferenceParticipantUnsetAdmin,
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		addr
	);
	return event;
}

shared_ptr<ConferenceSubjectEvent> LocalConferenceEventHandler::notifySubjectChanged () {
	L_D();
	d->notifyAll(d->createNotifySubjectChanged());
	shared_ptr<ConferenceSubjectEvent> event = make_shared<ConferenceSubjectEvent>(
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		d->conf->getSubject()
	);
	return event;
}

shared_ptr<ConferenceParticipantDeviceEvent> LocalConferenceEventHandler::notifyParticipantDeviceAdded (const Address &addr, const Address &gruu) {
	L_D();
	d->notifyAll(d->createNotifyParticipantDeviceAdded(addr, gruu));
	shared_ptr<ConferenceParticipantDeviceEvent> event = make_shared<ConferenceParticipantDeviceEvent>(
		EventLog::Type::ConferenceParticipantDeviceAdded,
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		addr,
		gruu
	);
	return event;
}

shared_ptr<ConferenceParticipantDeviceEvent> LocalConferenceEventHandler::notifyParticipantDeviceRemoved (const Address &addr, const Address &gruu) {
	L_D();
	d->notifyAll(d->createNotifyParticipantDeviceRemoved(addr, gruu));
	shared_ptr<ConferenceParticipantDeviceEvent> event = make_shared<ConferenceParticipantDeviceEvent>(
		EventLog::Type::ConferenceParticipantDeviceRemoved,
		time(nullptr),
		d->chatRoomId,
		d->lastNotify,
		addr,
		gruu
	);
	return event;
}

void LocalConferenceEventHandler::setLastNotify (unsigned int lastNotify) {
	L_D();
	d->lastNotify = lastNotify;
}

void LocalConferenceEventHandler::setChatRoomId (const ChatRoomId &chatRoomId) {
	L_D();
	d->chatRoomId = chatRoomId;
}

ChatRoomId LocalConferenceEventHandler::getChatRoomId () const {
	L_D();
	return d->chatRoomId;
}

string LocalConferenceEventHandler::getNotifyForId (int notifyId, bool oneToOne) {
	L_D();
	if (notifyId == 0)
		return d->createNotifyFullState(static_cast<int>(d->lastNotify), oneToOne);
	else if (notifyId < static_cast<int>(d->lastNotify))
		return d->createNotifyMultipart(notifyId);

	return Utils::getEmptyConstRefObject<string>();
}

LINPHONE_END_NAMESPACE
