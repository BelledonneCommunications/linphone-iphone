/*
 * remote-conference-event-handler.cpp
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

#include <sstream>

#include "linphone/utils/algorithm.h"
#include "linphone/utils/utils.h"

#include "conference/remote-conference.h"
#include "content/content-manager.h"
#include "content/content-type.h"
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

	IdentityAddress entityAddress(confInfo->getEntity().c_str());
	if (entityAddress != chatRoomId.getPeerAddress())
		return;

	auto &confDescription = confInfo->getConferenceDescription();

	// 1. Compute event time.
	time_t creationTime = time(nullptr);
	{
		auto &freeText = confDescription->getFreeText();
		if (freeText.present())
			creationTime = static_cast<time_t>(Utils::stoll(freeText.get()));
	}

	// 2. Update last notify.
	{
		auto &version = confInfo->getVersion();
		if (version.present())
			lastNotify = version.get();
	}

	bool isFullState = confInfo->getState() == StateType::full;
	ConferenceListener *confListener = static_cast<ConferenceListener *>(conf);

	// 3. Notify subject and keywords.
	if (confDescription.present()) {
		auto &subject = confDescription.get().getSubject();
		if (subject.present() && !subject.get().empty())
			confListener->onSubjectChanged(
				make_shared<ConferenceSubjectEvent>(
					creationTime,
					chatRoomId,
					lastNotify,
					subject.get()
				),
				isFullState
			);

		auto &keywords = confDescription.get().getKeywords();
		if (keywords.present() && !keywords.get().empty()) {
			KeywordsType xmlKeywords = keywords.get();
			confListener->onConferenceKeywordsChanged(
				vector<string>(xmlKeywords.begin(), xmlKeywords.end())
			);
		}
	}

	auto &users = confInfo->getUsers();
	if (!users.present())
		return;

	// 4. Notify changes on users.
	for (auto &user : users->getUser()) {
		LinphoneAddress *cAddr = linphone_core_interpret_url(conf->getCore()->getCCore(), user.getEntity()->c_str());
		char *cAddrStr = linphone_address_as_string(cAddr);
		linphone_address_unref(cAddr);

		Address addr(cAddrStr);
		bctbx_free(cAddrStr);

		StateType state = user.getState();

		if (state == StateType::deleted) {
			confListener->onParticipantRemoved(
				make_shared<ConferenceParticipantEvent>(
					EventLog::Type::ConferenceParticipantRemoved,
					creationTime,
					chatRoomId,
					lastNotify,
					addr
				),
				isFullState
			);

			continue;
		}

		if (state == StateType::full)
			confListener->onParticipantAdded(
				make_shared<ConferenceParticipantEvent>(
					EventLog::Type::ConferenceParticipantAdded,
					creationTime,
					chatRoomId,
					lastNotify,
					addr
				),
				isFullState
			);

		auto &roles = user.getRoles();
		if (roles) {
			auto &entry = roles->getEntry();
			confListener->onParticipantSetAdmin(
				make_shared<ConferenceParticipantEvent>(
					find(entry, "admin") != entry.end()
						? EventLog::Type::ConferenceParticipantSetAdmin
						: EventLog::Type::ConferenceParticipantUnsetAdmin,
					creationTime,
					chatRoomId,
					lastNotify,
					addr
				),
				isFullState
			);
		}

		for (const auto &endpoint : user.getEndpoint()) {
			if (!endpoint.getEntity().present())
				break;

			Address gruu(endpoint.getEntity().get());
			StateType state = endpoint.getState();

			if (state == StateType::deleted) {
				confListener->onParticipantDeviceRemoved(
					make_shared<ConferenceParticipantDeviceEvent>(
						EventLog::Type::ConferenceParticipantDeviceRemoved,
						creationTime,
						chatRoomId,
						lastNotify,
						addr,
						gruu
					),
					isFullState
				);
			} else if (state == StateType::full) {
				confListener->onParticipantDeviceAdded(
					make_shared<ConferenceParticipantDeviceEvent>(
						EventLog::Type::ConferenceParticipantDeviceAdded,
						creationTime,
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

	if (isFullState)
		confListener->onFirstNotifyReceived(chatRoomId.getPeerAddress());
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandlerPrivate::subscribe () {
	if (lev || !subscriptionWanted)
		return; // Already subscribed or application did not request subscription

	const string &peerAddress = chatRoomId.getPeerAddress().asString();
	LinphoneAddress *lAddr = linphone_address_new(peerAddress.c_str());
	LinphoneCore *lc = conf->getCore()->getCCore();
	LinphoneProxyConfig *cfg = linphone_core_lookup_known_proxy(lc, lAddr);
	if (!cfg || (linphone_proxy_config_get_state(cfg) != LinphoneRegistrationOk)) {
		linphone_address_unref(lAddr);
		return;
	}

	lev = linphone_core_create_subscribe(conf->getCore()->getCCore(), lAddr, "conference", 600);
	lev->op->setFrom(chatRoomId.getLocalAddress().asString().c_str());
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

void RemoteConferenceEventHandlerPrivate::onNetworkReachable (bool sipNetworkReachable, bool mediaNetworkReachable) {
	if (!sipNetworkReachable)
		unsubscribe();
}

void RemoteConferenceEventHandlerPrivate::onRegistrationStateChanged (LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const std::string &message) {
	if (state == LinphoneRegistrationOk)
		subscribe();
}

void RemoteConferenceEventHandlerPrivate::onEnteringBackground () {
	unsubscribe();
}

void RemoteConferenceEventHandlerPrivate::onEnteringForeground () {
	subscribe();
}

// -----------------------------------------------------------------------------

RemoteConferenceEventHandler::RemoteConferenceEventHandler (RemoteConference *remoteConference) :
Object(*new RemoteConferenceEventHandlerPrivate) {
	L_D();
	d->conf = remoteConference;
	d->conf->getCore()->getPrivate()->registerListener(d);
}

RemoteConferenceEventHandler::~RemoteConferenceEventHandler () {
	L_D();

	try {
		d->conf->getCore()->getPrivate()->unregisterListener(d);
	} catch (const bad_weak_ptr &) {
		// Unable to unregister listener here. Core is destroyed and the listener doesn't exist.
	}

	unsubscribe();
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

	lInfo() << "NOTIFY received for conference: " << d->chatRoomId;

	d->simpleNotifyReceived(xmlBody);
}

void RemoteConferenceEventHandler::multipartNotifyReceived (const string &xmlBody) {
	L_D();

	lInfo() << "multipart NOTIFY received for conference: " << d->chatRoomId;

	Content multipart;
	multipart.setBody(xmlBody);
	ContentType contentType(ContentType::Multipart);
	contentType.addParameter("boundary", MultipartBoundary);
	multipart.setContentType(contentType);

	for (const auto &content : ContentManager::multipartToContentList(multipart))
		d->simpleNotifyReceived(content.getBodyAsString());
}

// -----------------------------------------------------------------------------

void RemoteConferenceEventHandler::setChatRoomId (ChatRoomId chatRoomId) {
	L_D();
	d->chatRoomId = chatRoomId;
}

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
