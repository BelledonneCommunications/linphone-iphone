/*
 * remote-conference-event-handler.cpp
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

#include "linphone/utils/utils.h"

#include "conference/remote-conference.h"
#include "content/content-manager.h"
#include "content/content.h"
#include "core/core-p.h"
#include "logger/logger.h"
#include "remote-conference-event-handler-p.h"
#include "xml/conference-info.h"

// TODO: Remove me later.
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

using namespace Xsd::ConferenceInfo;

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandlerPrivate::simpleNotifyReceived (const string &xmlBody) {
	istringstream data(xmlBody);
	unique_ptr<ConferenceType> confInfo = parseConferenceInfo(data, Xsd::XmlSchema::Flags::dont_validate);
	time_t tm = time(nullptr);
	if (confInfo->getConferenceDescription()->getFreeText().present())
		tm = static_cast<time_t>(Utils::stoll(confInfo->getConferenceDescription()->getFreeText().get()));

	bool isFullState = (confInfo->getState() == StateType::full);

	ConferenceListener *confListener = static_cast<ConferenceListener *>(conf);

	IdentityAddress entityAddress(confInfo->getEntity().c_str());
	if (entityAddress == chatRoomId.getPeerAddress()) {
		if (
			confInfo->getConferenceDescription().present() &&
			confInfo->getConferenceDescription().get().getSubject().present() &&
			!confInfo->getConferenceDescription().get().getSubject().get().empty()
		)
			confListener->onSubjectChanged(
				make_shared<ConferenceSubjectEvent>(
					tm,
					chatRoomId,
					lastNotify,
					confInfo->getConferenceDescription().get().getSubject().get()
				),
				isFullState
			);
		if (confInfo->getVersion().present())
			lastNotify = confInfo->getVersion().get();

		if (!confInfo->getUsers().present())
			return;

		for (const auto &user : confInfo->getUsers()->getUser()) {
			LinphoneAddress *cAddr = linphone_core_interpret_url(conf->getCore()->getCCore(), user.getEntity()->c_str());
			char *cAddrStr = linphone_address_as_string(cAddr);
			Address addr(cAddrStr);
			bctbx_free(cAddrStr);
			if (user.getState() == StateType::deleted) {
				confListener->onParticipantRemoved(
					make_shared<ConferenceParticipantEvent>(
						EventLog::Type::ConferenceParticipantRemoved,
						tm,
						chatRoomId,
						lastNotify,
						addr
					),
					isFullState
				);
			} else {
				bool isAdmin = false;
				if (user.getRoles()) {
					for (const auto &entry : user.getRoles()->getEntry()) {
						if (entry == "admin") {
							isAdmin = true;
							break;
						}
					}
				}

				if (user.getState() == StateType::full) {
					confListener->onParticipantAdded(
						make_shared<ConferenceParticipantEvent>(
							EventLog::Type::ConferenceParticipantAdded,
							tm,
							chatRoomId,
							lastNotify,
							addr
						),
						isFullState
					);
				}

				confListener->onParticipantSetAdmin(
					make_shared<ConferenceParticipantEvent>(
						isAdmin ? EventLog::Type::ConferenceParticipantSetAdmin : EventLog::Type::ConferenceParticipantUnsetAdmin,
						tm,
						chatRoomId,
						lastNotify,
						addr
					),
					isFullState
				);

				for (const auto &endpoint : user.getEndpoint()) {
					if (!endpoint.getEntity().present())
						break;

					Address gruu(endpoint.getEntity().get());
					if (endpoint.getState() == StateType::deleted) {
						confListener->onParticipantDeviceRemoved(
							make_shared<ConferenceParticipantDeviceEvent>(
								EventLog::Type::ConferenceParticipantDeviceRemoved,
								tm,
								chatRoomId,
								lastNotify,
								addr,
								gruu
							),
							isFullState
						);
					} else if (endpoint.getState() == StateType::full) {
						confListener->onParticipantDeviceAdded(
							make_shared<ConferenceParticipantDeviceEvent>(
								EventLog::Type::ConferenceParticipantDeviceAdded,
								tm,
								chatRoomId,
								lastNotify,
								addr,
								gruu
							),
							isFullState
						);
					}
				}
			}
			linphone_address_unref(cAddr);
		}

		if (isFullState)
			confListener->onFirstNotifyReceived(chatRoomId.getPeerAddress());
	}
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandlerPrivate::subscribe () {
	if (lev || !subscriptionWanted)
		return; // Already subscribed or application did not request subscription

	const string &peerAddress = chatRoomId.getPeerAddress().asString();
	LinphoneAddress *lAddr = linphone_address_new(peerAddress.c_str());
	LinphoneCore *lc = conf->getCore()->getCCore();
	LinphoneProxyConfig *cfg = linphone_core_lookup_known_proxy(lc, lAddr);
	if (!cfg || (linphone_proxy_config_get_state(cfg) != LinphoneRegistrationOk))
		return;

	lev = linphone_core_create_subscribe(conf->getCore()->getCCore(), lAddr, "conference", 600);
	lev->op->set_from(chatRoomId.getLocalAddress().asString().c_str());
	const string &lastNotifyStr = Utils::toString(lastNotify);
	linphone_event_add_custom_header(lev, "Last-Notify-Version", lastNotifyStr.c_str());
	linphone_address_unref(lAddr);
	linphone_event_set_internal(lev, TRUE);
	linphone_event_set_user_data(lev, this);
	lInfo() << "Subscribing to chat room: " << peerAddress << "with last notify: " << lastNotifyStr;
	linphone_event_send_subscribe(lev, nullptr);
}

void RemoteConferenceEventHandlerPrivate::unsubscribe () {
	if (lev) {
		linphone_event_terminate(lev);
		lev = nullptr;
	}
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandlerPrivate::onNetworkReachable (bool reachable) {
	if (!reachable)
		unsubscribe();
}

void RemoteConferenceEventHandlerPrivate::onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message) {
	if (state == LinphoneRegistrationOk)
		subscribe();
}

// -----------------------------------------------------------------------------

RemoteConferenceEventHandler::RemoteConferenceEventHandler (RemoteConference *remoteConference) :
Object(*new RemoteConferenceEventHandlerPrivate) {
	L_D();
	xercesc::XMLPlatformUtils::Initialize();
	d->conf = remoteConference;
	d->conf->getCore()->getPrivate()->registerListener(d);
}

RemoteConferenceEventHandler::~RemoteConferenceEventHandler () {
	L_D();
	d->conf->getCore()->getPrivate()->unregisterListener(d);
	xercesc::XMLPlatformUtils::Terminate();
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandler::subscribe (const ChatRoomId &chatRoomId) {
	L_D();
	d->chatRoomId = chatRoomId;
	d->subscriptionWanted = true;
	d->subscribe();
}

void RemoteConferenceEventHandler::unsubscribe () {
	L_D();
	d->unsubscribe();
	d->subscriptionWanted = false;
}

void RemoteConferenceEventHandler::notifyReceived (const string &xmlBody) {
	L_D();

	lInfo() << "NOTIFY received for conference: (remote=" << d->chatRoomId.getPeerAddress().asString() <<
		", local=" << d->chatRoomId.getLocalAddress().asString() << ").";

	d->simpleNotifyReceived(xmlBody);
}

void RemoteConferenceEventHandler::multipartNotifyReceived (const string &xmlBody) {
	L_D();

	lInfo() << "multipart NOTIFY received for conference: (remote=" << d->chatRoomId.getPeerAddress().asString() <<
		", local=" << d->chatRoomId.getLocalAddress().asString() << ").";

	Content multipart;
	multipart.setBody(xmlBody);

	for (const auto &content : ContentManager::multipartToContentList(multipart))
		d->simpleNotifyReceived(content.getBodyAsString());
}

// -----------------------------------------------------------------------------

const ChatRoomId &RemoteConferenceEventHandler::getChatRoomId () const {
	L_D();
	return d->chatRoomId;
}

unsigned int RemoteConferenceEventHandler::getLastNotify () const {
	L_D();
	return d->lastNotify;
};

void RemoteConferenceEventHandler::setLastNotify (unsigned int lastNotify) {
	L_D();
	d->lastNotify = lastNotify;
}

void RemoteConferenceEventHandler::resetLastNotify () {
	setLastNotify(0);
}

LINPHONE_END_NAMESPACE
